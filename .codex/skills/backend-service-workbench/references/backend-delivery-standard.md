# Backend Delivery Standard

Usar esta referencia para manter a entrega de backend consistente, testavel e segura.

## Fluxo padrao

1. Inspecionar stack, contratos e docs existentes.
2. Delimitar o tipo de mudanca.
3. Implementar na estrutura correta.
4. Atualizar validacao, erros, persistencia e seguranca.
5. Adicionar ou ajustar testes.
6. Validar e documentar a entrega.

## Diretriz de arquitetura para MVP

- Para backends Python simples, preferir monolito modular enxuto.
- Estrutura sugerida:
  - `main.py`, `dependencies.py`, `routes.py` e `schemas.py` para a borda HTTP
  - `service.py` para regra de negocio
  - `repository.py` para persistencia
  - `crypto.py`, `qr_reader.py` e `totp.py` para integracoes concretas
  - `config.py` para configuracao
- So escalar para arquitetura em camadas completas quando houver necessidade real e explicitada.

## Tipos de mudanca e checklist

### Endpoint novo ou alterado

- Definir entrada, saida e codigos de status.
- Validar payload e parametros.
- Garantir tratamento de erro coerente.
- Garantir que regra de negocio nao fique espalhada no controller.
- Atualizar docs e exemplos de uso.
- Adicionar teste de contrato, integracao ou smoke apropriado.

### Regra de negocio

- Isolar a regra em servico ou modulo de negocio apropriado.
- Cobrir caminho feliz e bordas principais.
- Evitar depender de transporte HTTP diretamente.
- Revisar impacto em contratos existentes.

### Persistencia ou schema

- Explicitar mudanca de schema, migracao e rollback quando aplicavel.
- Atualizar repositorios, models ou mapeamentos.
- Testar leitura, escrita e compatibilidade.
- Destacar riscos de dados e backward compatibility.

### Autenticacao e autorizacao

- Cobrir sucesso, falha e negacao de acesso.
- Validar expiracao, credenciais e permissoes.
- Evitar logar segredos.
- Documentar requisitos de configuracao.

### Integracao externa

- Isolar cliente ou gateway.
- Tratar timeout, retry e falha externa.
- Mapear erros externos para erros internos coerentes.
- Simular ou mockar integracoes nos testes quando necessario.

## Checklist final de entrega

- A mudanca ficou no modulo correto.
- O contrato da API ou do backend foi respeitado ou atualizado.
- Os testes relevantes passaram.
- A documentacao necessaria foi atualizada.
- Riscos e lacunas remanescentes foram declarados.
