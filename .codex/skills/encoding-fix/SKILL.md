---
name: encoding-fix
description: Encontrar e corrigir automaticamente problemas de encoding e mojibake em repositorios (acentos quebrados, caracteres estranhos, texto corrompido), padronizando arquivos de texto para UTF-8 sem BOM com seguranca e sem alterar semantica. Usar quando o usuario pedir mojibake, encoding errado, acentos quebrados, caracteres estranhos, corrigir encoding, unicode, utf-8 ou texto corrompido.
---

# Encoding Fix

## Objetivo

Detectar e corrigir encoding quebrado em arquivos de texto do projeto inteiro, mantendo o comportamento da aplicacao e registrando relatorio datado local.
Nao encerrar em diagnostico: corrigir, validar e reportar.

## Regras obrigatorias

1. Analisar o projeto como um todo (codigo, markdown, configs, json/yaml e demais textos).
2. Ignorar binarios e pastas de build/cache/dependencias externas (`.git`, `node_modules`, `dist`, `build`, `.venv`, `venv`, etc).
3. Padronizar texto para UTF-8 sem BOM, salvo excecao explicita do proprio repo.
4. Manter EOL conforme o repo; nao normalizar finais de linha sem regra clara.
5. Corrigir apenas encoding/mojibake; nao reescrever frases, nao ajustar ortografia, nao renomear simbolos.
6. Evitar alterar arquivos gerados/vendorizados; se houver risco, pular e justificar no relatorio.
7. Rodar validacoes reais do repo (lint/test/build, quando existirem) sem inventar comandos.

## Script da skill

Preferir o script bundled para varredura/correcao em lote:

`scripts/encoding_fix.py`

Exemplo de uso:

```bash
python <skill-dir>/scripts/encoding_fix.py --root . --write --json
```

O script cobre:

1. Deteccao de BOM UTF-8
2. Deteccao de bytes invalidos para UTF-8
3. Conversao de arquivos em `cp1252`/`latin1` para UTF-8
4. Reparo de padroes comuns de mojibake (`Ã§`, `Ã£`, `Ã¡`, `â€“`, `â€œ`, `â€`, `�`, etc.)
5. Saida estruturada (incluindo arquivos corrigidos, tipo de problema e riscos/pulos)

## Processo obrigatorio

1. Detectar stack/padroes do repo (`.editorconfig`, configs de formatter/linter, convencoes locais).
2. Descobrir comandos reais de validacao no repo (`package.json`, `Makefile`, `pyproject.toml`, scripts).
3. Rodar varredura de encoding/mojibake em arquivos de texto.
4. Corrigir em lotes pequenos:
   - converter para UTF-8 sem BOM
   - reparar caracteres corrompidos por encoding
5. Revalidar com comandos existentes (lint/test/build quando houver).
6. Se alguma validacao quebrar, corrigir imediatamente.
7. Gerar relatorio datado local em `encoding_reports/`.
8. Nao versionar nem commitar arquivos de `encoding_reports/`.

## Relatorio datado obrigatorio

1. Garantir pasta `encoding_reports/` na raiz.
2. Garantir que `encoding_reports/` esteja no exclude local do Git (`.git/info/exclude`), sem orientar mudancas no `.gitignore` do repositorio:

```text
encoding_reports/
```

3. Gerar um arquivo por execucao:
`encoding_reports/YYYY-MM-DD_HH-mm_encoding-fix.md`

4. Incluir obrigatoriamente:
- Escopo (pastas varridas e pastas ignoradas)
- Lista de arquivos corrigidos (antes/depois: encoding detectado -> UTF-8)
- Tipo de problema por arquivo (BOM, bytes invalidos, mojibake, cp1252/latin1)
- Observacoes de risco (arquivos pulados e justificativa)
- Comandos executados (lint/test/build) e resultado
- Lista de arquivos alterados

Tratar esses relatórios como artefatos locais apenas.

## Formato de saida obrigatorio no chat

Sempre responder com:

1. Quantidade de arquivos corrigidos
2. Lista dos arquivos principais corrigidos
3. Comandos de validacao rodados (se existirem)
4. Nome do arquivo gerado em `encoding_reports/`
