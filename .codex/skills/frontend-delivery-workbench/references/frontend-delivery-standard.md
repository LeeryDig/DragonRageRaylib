# Frontend Delivery Standard

Usar esta referencia para manter entregas de frontend consistentes, usaveis e prontas para validacao.

## Fluxo padrao

1. Inspecionar stack, arquitetura e docs existentes.
2. Delimitar o tipo de mudanca.
3. Implementar no modulo correto.
4. Validar UX, estados e integracoes.
5. Atualizar testes e documentacao.
6. Verificar build, responsividade e prontidao final.

## Tipos de mudanca e checklist

### Componente novo ou alterado

- Definir claramente responsabilidade e nivel de reutilizacao.
- Cobrir variantes, estados e props importantes.
- Evitar logica de negocio excessiva dentro do componente visual.
- Revisar acessibilidade basica e responsividade.

### Pagina ou fluxo

- Garantir loading, empty, error e success states quando aplicavel.
- Validar navegacao, CTA principal e hierarquia visual.
- Garantir integracao coerente com roteamento e layout.
- Verificar experiencia em desktop e mobile.

### Formulario

- Validar campos, mensagens, submit e erro.
- Cobrir estado de envio e prevencao de submit duplo.
- Garantir feedback claro de sucesso ou falha.
- Revisar acessibilidade de labels, foco e leitura.

### Consumo de API

- Isolar chamadas remotas em servicos, hooks ou camada equivalente.
- Tratar loading, retry, erro e estado vazio.
- Evitar fetch espalhado em qualquer componente sem criterio.
- Verificar compatibilidade com contrato da API.

### Estado global ou compartilhado

- Justificar por que o estado precisa ser compartilhado.
- Evitar store global sem dono claro.
- Garantir que seletores, actions ou hooks fiquem previsiveis.
- Testar o fluxo principal impactado.

## Checklist final de entrega

- A mudanca ficou no modulo correto.
- A arquitetura do frontend foi respeitada ou atualizada conscientemente.
- Responsividade basica foi verificada.
- Acessibilidade basica foi considerada.
- Testes, lint, type-check ou build relevantes foram executados quando disponiveis.
- Riscos e gaps remanescentes foram declarados.
