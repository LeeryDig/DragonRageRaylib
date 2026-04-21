# Git Safety Checklist

Usar esta referencia antes de publicar qualquer entrega.

## Antes de `git add`

- o escopo da entrega esta claro
- os arquivos alterados pertencem ao escopo
- nao existem artefatos locais que nao devem entrar no commit
- a pasta `delivery-audits/` esta fora do versionamento

## Antes de `git commit`

- a auditoria local foi escrita
- a mensagem de commit descreve o diff real
- os testes ou validacoes relevantes foram registrados
- o stage nao contem mudancas acidentais

## Antes de `git push`

- remote e branch estao corretos
- o usuario confirmou a publicacao
- nao ha necessidade de `--force`
- o relatorio sera atualizado com o SHA final
