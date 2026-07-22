# Créditos

O MangaDex Reader é construído em cima do trabalho de várias pessoas e
projetos de código aberto. Nenhum deles tem qualquer afiliação com este
projeto - só estamos agradecendo e cumprindo as licenças de cada um.

## Bibliotecas e ferramentas

- **[Borealis](https://github.com/natinusala/borealis)** por natinusala e
  colaboradores — a biblioteca de interface gráfica usada em todo o app
  (menus, abas, leitor, teclado virtual). Licença Apache 2.0. Incluída como
  submódulo git em `external/borealis`.
- **[Yoga](https://github.com/facebook/yoga)** (Meta) — motor de layout
  flexbox usado internamente pelo Borealis. Licença MIT.
- **[nanovg](https://github.com/memononen/nanovg)** por Mikko Mononen —
  biblioteca de desenho vetorial 2D usada internamente pelo Borealis pra
  renderizar tudo na tela. Licença zlib.
- **[deko3d](https://github.com/devkitPro/deko3d)** e **[libnx](https://github.com/switchbrew/libnx)**
  (devkitPro / switchbrew) — API gráfica de baixo nível e biblioteca de
  sistema do Switch usadas pelo Borealis e por este app. Licença zlib/ISC.
- **[devkitPro / devkitA64](https://devkitpro.org/)** — toolchain de
  compilação cruzada usada pra gerar o binário do Switch.
- **[curl](https://curl.se/)** por Daniel Stenberg e colaboradores —
  cliente HTTP usado pra toda comunicação com a API do MangaDex. Licença
  curl (estilo MIT).
- **[mbedTLS](https://www.trustedfirmware.org/projects/mbed-tls/)** (Arm) —
  biblioteca de TLS usada pelo curl pra HTTPS. Este projeto usa uma build
  própria (mbedTLS 3.6.2) porque a versão padrão do devkitPro não suporta
  TLS 1.3, exigido pelos servidores da MangaDex. Licença Apache 2.0.
- **[nlohmann/json](https://github.com/nlohmann/json)** por Niels Lohmann —
  biblioteca de parsing de JSON usada pra ler as respostas da API. Licença
  MIT. Incluída como header único em `external/json.hpp`.
- **[Mozilla CA Certificate Store](https://curl.se/docs/caextract.html)** —
  pacote de certificados raiz (`cacert.pem`) usado pra validar as conexões
  HTTPS, distribuído pelo próprio projeto curl a partir do repositório de
  certificados confiáveis da Mozilla. Licença MPL 2.0.

## Fonte de conteúdo

- **[MangaDex](https://mangadex.org/)** e sua
  **[API pública](https://api.mangadex.org/docs/)** — toda busca, listagem e
  leitura de mangá neste app vem exclusivamente da API pública e gratuita da
  MangaDex. Este projeto não é afiliado, endossado ou associado à MangaDex
  de nenhuma forma; é um cliente feito por fãs, usando a API do jeito que
  ela foi documentada e disponibilizada publicamente pela própria MangaDex
  para uso por terceiros.

## Agradecimentos

- **CostelaBR**
- **AurelioEB**
- **Claude** (Anthropic) — usado como copiloto de programação durante todo
  o desenvolvimento: da arquitetura inicial até a depuração de bugs reais
  encontrados só em hardware (incluindo dois bugs genuínos na própria
  biblioteca Borealis, corrigidos diretamente no submódulo vendorizado
  deste projeto). Todo o código foi escrito, revisado e testado em conjunto
  com o autor humano.
