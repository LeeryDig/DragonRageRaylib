const http = require('http');
const dgram = require('dgram');
const { spawn, spawnSync } = require('child_process');
const ytdl = require('@distube/ytdl-core');
const ffmpeg = require('fluent-ffmpeg');
const ffmpegStatic = require('ffmpeg-static');

if (ffmpegStatic) ffmpeg.setFfmpegPath(ffmpegStatic);

const HOST = '127.0.0.1';
const PORT = Number(process.env.PORT || 3456);
const SAMPLE_RATE = 44100;
const BYTES_PER_SECOND = SAMPLE_RATE * 2; // s16le mono: 2 bytes por sample

let current = null;

function stopCurrent() {
  if (!current) return;
  try { current.ffmpeg && current.ffmpeg.kill('SIGKILL'); } catch (_) {}
  try { current.inputProcess && current.inputProcess.kill('SIGKILL'); } catch (_) {}
  try { current.input && current.input.destroy(); } catch (_) {}
  try { current.timers && current.timers.forEach((timer) => clearTimeout(timer)); } catch (_) {}
  try { current.udp && current.udp.close(); } catch (_) {}
  current = null;
}

function send(res, code, data) {
  res.writeHead(code, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*' });
  res.end(JSON.stringify(data));
}

function createYoutubeInput(url) {
  // yt-dlp costuma ser mais robusto contra mudanças do YouTube que ytdl-core.
  const hasYtDlp = spawnSync('yt-dlp', ['--version'], { stdio: 'ignore' }).status === 0;
  if (hasYtDlp) {
    const ytdlp = spawn('yt-dlp', ['-f', 'bestaudio/best', '-o', '-', '--no-playlist', url], {
      stdio: ['ignore', 'pipe', 'pipe']
    });
    ytdlp.stderr.on('data', (chunk) => console.error('[youtube-audio] yt-dlp:', chunk.toString().trim()));
    ytdlp.on('close', (code) => { if (code !== 0) console.error(`[youtube-audio] yt-dlp exited with code ${code}`); });
    ytdlp.stdout._dragonrageProcess = ytdlp;
    return ytdlp.stdout;
  }

  console.warn('[youtube-audio] yt-dlp not found in PATH; falling back to @distube/ytdl-core');
  const input = ytdl(url, { quality: 'highestaudio', filter: 'audioonly', highWaterMark: 1 << 25 });
  input.on('error', (err) => {
    console.error('[youtube-audio] ytdl error:', err.message);
    stopCurrent();
  });
  return input;
}

function startPlay(url, udpPort) {
  stopCurrent();
  const udp = dgram.createSocket('udp4');
  const input = createYoutubeInput(url);
  const timers = [];
  let scheduledBytes = 0;
  const playbackStartMs = Date.now() + 350; // pequeno pré-buffer para evitar cortes

  const command = ffmpeg(input)
    .noVideo()
    .audioCodec('pcm_s16le')
    .audioChannels(1)
    .audioFrequency(SAMPLE_RATE)
    .format('s16le')
    .on('error', (err) => {
      console.error('[youtube-audio] ffmpeg error:', err.message);
      stopCurrent();
    })
    .on('end', () => console.log('[youtube-audio] stream ended'));

  const out = command.pipe();
  out.on('data', (chunk) => {
    for (let offset = 0; offset < chunk.length; offset += 1200) {
      const packet = Buffer.from(chunk.subarray(offset, Math.min(offset + 1200, chunk.length)));
      const sendAtMs = playbackStartMs + (scheduledBytes / BYTES_PER_SECOND) * 1000;
      scheduledBytes += packet.length;
      const delayMs = Math.max(0, sendAtMs - Date.now());
      const timer = setTimeout(() => {
        udp.send(packet, udpPort, HOST, (err) => {
          if (err) console.error('[youtube-audio] udp send error:', err.message);
        });
      }, delayMs);
      timers.push(timer);
    }
  });
  out.on('error', (err) => console.error('[youtube-audio] pipe error:', err.message));

  current = { url, udpPort, udp, input, inputProcess: input._dragonrageProcess, ffmpeg: command, timers };
}

const server = http.createServer((req, res) => {
  const parsed = new URL(req.url, `http://${req.headers.host}`);
  if (parsed.pathname === '/health') return send(res, 200, { ok: true, playing: !!current });
  if (parsed.pathname === '/stop') { stopCurrent(); return send(res, 200, { ok: true }); }
  if (parsed.pathname === '/play') {
    const url = parsed.searchParams.get('url') || '';
    const udpPort = Number(parsed.searchParams.get('port') || 0);
    if (!url || !udpPort) return send(res, 400, { ok: false, error: 'Missing url or port' });
    try { startPlay(url, udpPort); return send(res, 200, { ok: true, port: udpPort }); }
    catch (err) { stopCurrent(); return send(res, 500, { ok: false, error: err.message }); }
  }
  send(res, 404, { ok: false, error: 'Not found' });
});

server.listen(PORT, HOST, () => console.log(`[youtube-audio] listening on http://${HOST}:${PORT}`));
process.on('SIGINT', () => { stopCurrent(); process.exit(0); });
