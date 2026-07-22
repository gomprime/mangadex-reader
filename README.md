# MangaDex Reader

Um leitor de mangás homebrew para Nintendo Switch, usando a
[API pública da MangaDex](https://api.mangadex.org/docs/) como fonte de
conteúdo. Feito com [Borealis](https://github.com/natinusala/borealis) e
[libnx](https://github.com/switchbrew/libnx)/[deko3d](https://github.com/devkitPro/deko3d).

Não afiliado à MangaDex, à Nintendo ou a qualquer editora. Projeto de fã,
sem fins lucrativos.

## O que este app faz (e o que não faz)

Este app só busca, lista e exibe capítulos que estão **hospedados de
verdade na MangaDex** e acessíveis pela API pública dela
(`/manga`, `/manga/{id}/feed`, `/at-home/server/{chapterId}`).

Ele **não** acessa, abre, faz scraping ou de qualquer forma segue os links
externos que a MangaDex às vezes lista para leitores oficiais/licenciados
de terceiros (por exemplo o MangaPlus, quando uma editora pede remoção do
conteúdo fã e só sobra o link para o site oficial). Quando um capítulo só
existe como link externo, o app mostra isso claramente na lista (em cinza,
sem poder abrir) em vez de tentar carregá-lo.

## Instalação

Requer um Switch com Atmosphère (ou outro CFW com suporte a homebrew) já
configurado.

1. Baixe o `mangadex-reader.nro` da [página de
   Releases](https://github.com/gomprime/mangadex-reader/releases/latest).
2. Copie o arquivo para `sdmc:/switch/mangadex-reader/mangadex-reader.nro`
   no cartão SD do console (crie as pastas `switch/mangadex-reader/` se
   ainda não existirem).
3. Abra o Album Homebrew Menu (hbmenu) no console e selecione
   "MangaDex Reader" na lista.

## Funcionalidades

- Busca por título, navegação por novidades/populares
- Biblioteca local (favoritos + progresso de leitura), tudo salvo só no
  cartão SD - sem conta, sem login, sem sincronização externa
- Leitor com zoom e pan pelo analógico esquerdo
- Filtro de idioma configurável (busca, novidades e capítulos)
- Filtro de classificação de conteúdo
- Cache de páginas/capas em disco com limite configurável

## Compilando

Requer [devkitPro](https://devkitpro.org/) com devkitA64 e libnx instalados.

```sh
git clone --recurse-submodules <url-deste-repositório>
cd mangadex-reader
export DEVKITPRO=/opt/devkitpro   # ajuste pro seu caminho
make
```

Gera `mangadex-reader.nro` — veja a seção [Instalação](#instalação) acima
pra colocá-lo no cartão SD.

## Créditos

Veja [CREDITS.md](CREDITS.md) para a lista completa de bibliotecas,
ferramentas e pessoas envolvidas — incluindo o crédito à API da MangaDex,
que é quem torna esse app possível.

## Licença

Código original deste projeto sob licença MIT — veja [LICENSE](LICENSE).
Componentes de terceiros vendorizados mantêm suas próprias licenças (ver
CREDITS.md).
