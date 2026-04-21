# Unit Test Guidelines

## Quando usar teste unitario

- regressao de servico
- validacao de endpoint sem browser real
- serializacao e respostas HTTP com `TestClient`
- regras de negocio e erros esperados
- adaptadores pequenos com fake ou stub controlado

## Quando nao usar teste unitario

- fluxo visual
- navegacao real
- verificacao de comportamento de interface
- criterio de aceite ponta a ponta

Nesses casos, usar Playwright.

## Heuristicas para isolamento

- preferir override de dependencia em FastAPI
- preferir fake em memoria a banco real
- controlar entradas e saidas explicitamente
- evitar tempo real, rede real e browser real

## Checklist final

- existe pelo menos um caminho feliz
- existe pelo menos uma falha importante
- o teste e determinista
- o comando foi executado localmente
- o resultado foi reportado no fechamento
