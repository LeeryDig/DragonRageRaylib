import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import { Type } from "typebox";
import { existsSync, readFileSync } from "node:fs";
import { join } from "node:path";

const AGENTS = [
  "orchestrator",
  "physics",
  "render",
  "assets",
  "gameplay",
  "editor-debug",
  "build-quality",
] as const;

type AgentName = (typeof AGENTS)[number];

function readAgentPrompt(cwd: string, agent: AgentName): string {
  const path = join(cwd, ".pi", "prompts", `agent-${agent}.md`);
  if (!existsSync(path)) return `Você é o agente ${agent}.`;
  return readFileSync(path, "utf8");
}

function buildAgentMessage(cwd: string, agent: AgentName, task: string, context?: string): string {
  const profile = readAgentPrompt(cwd, agent);
  return `${profile}

# Execução por orquestrador

Tarefa atual:
${task}

${context ? `Contexto/plano anterior:\n${context}\n` : ""}

Regras de orquestração automática:
- Execute sua etapa até o fim usando ferramentas normais do pi.
- Pode editar arquivos conforme seu escopo.
- Ao terminar, se precisar de outro domínio, chame a ferramenta \`handoff_agent\` com agente e próxima tarefa.
- Se tudo terminou, chame \`finish_orchestration\` com resumo final.
- Se encontrar risco alto ou ambiguidade, pare e explique no chat sem chamar outro agente.
`;
}

export default function (pi: ExtensionAPI) {
  let running = false;
  let lastTask = "";

  pi.registerCommand("orchestrate", {
    description: "Orquestra agentes do projeto em sequência automática",
    handler: async (args, ctx) => {
      const task = args.trim();
      if (!task) {
        ctx.ui.notify("Uso: /orchestrate <tarefa>", "warning");
        return;
      }
      if (!ctx.isIdle()) {
        ctx.ui.notify("Agente ocupado. Rode /orchestrate quando idle.", "warning");
        return;
      }

      running = true;
      lastTask = task;
      pi.setSessionName(`orchestrate: ${task.slice(0, 60)}`);
      ctx.ui.notify("Orquestração iniciada", "info");

      const msg = `${readAgentPrompt(ctx.cwd, "orchestrator")}

# Modo orquestração automática

Pedido do usuário:
${task}

Você deve resolver, não só planejar.

Fluxo obrigatório:
1. Entenda o pedido do usuário e leia somente os arquivos/docs necessários para essa tarefa.
2. Identifique domínios/agentes necessários.
3. Escolha primeiro agente executor.
4. Chame a ferramenta \`handoff_agent\` para iniciar esse agente.
5. Depois cada agente pode chamar o próximo via \`handoff_agent\`.
6. Quando tudo estiver pronto, algum agente deve chamar \`finish_orchestration\`.

Agentes disponíveis:
- physics
- render
- assets
- gameplay
- editor-debug
- build-quality

Não peça para o usuário chamar agentes manualmente. Você chama via ferramenta.`;

      pi.sendUserMessage(msg);
    },
  });

  pi.registerTool({
    name: "handoff_agent",
    label: "Handoff Agent",
    description: "Chama o próximo agente especializado da orquestração automática.",
    promptSnippet: "Chama próximo agente especializado numa orquestração sequencial.",
    promptGuidelines: [
      "Use handoff_agent quando uma tarefa orquestrada precisar passar para outro agente especializado.",
    ],
    parameters: Type.Object({
      agent: Type.Union(AGENTS.filter((a) => a !== "orchestrator").map((a) => Type.Literal(a))),
      task: Type.String({ description: "Tarefa específica para o próximo agente" }),
      context: Type.Optional(Type.String({ description: "Resumo do que já foi feito/plano/contratos" })),
    }),
    async execute(_toolCallId, params, _signal, _onUpdate, ctx) {
      if (!running) {
        return { content: [{ type: "text", text: "Nenhuma orquestração ativa." }], isError: true };
      }
      const agent = params.agent as AgentName;
      const message = buildAgentMessage(ctx.cwd, agent, params.task, params.context ?? `Pedido original: ${lastTask}`);
      pi.sendUserMessage(message, { deliverAs: "followUp" });
      return {
        content: [{ type: "text", text: `Agente ${agent} enfileirado como próxima etapa.` }],
        details: { agent, task: params.task },
      };
    },
  });

  pi.registerTool({
    name: "finish_orchestration",
    label: "Finish Orchestration",
    description: "Finaliza uma orquestração automática com resumo final.",
    promptSnippet: "Finaliza orquestração automática após validação.",
    promptGuidelines: [
      "Use finish_orchestration quando a tarefa orquestrada estiver concluída ou quando precisar parar com resumo claro.",
    ],
    parameters: Type.Object({
      summary: Type.String(),
      filesChanged: Type.Optional(Type.Array(Type.String())),
      validation: Type.Optional(Type.String()),
      risks: Type.Optional(Type.String()),
    }),
    async execute(_toolCallId, params) {
      running = false;
      return {
        content: [
          {
            type: "text",
            text: `Orquestração finalizada.\n\nResumo:\n${params.summary}\n\nArquivos:\n${(params.filesChanged ?? []).join("\n") || "(não informado)"}\n\nValidação:\n${params.validation ?? "(não informado)"}\n\nRiscos:\n${params.risks ?? "(não informado)"}`,
          },
        ],
        details: params,
      };
    },
  });
}
