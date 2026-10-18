
name: Build Dark Bot PSP

on:
  push:
  workflow_dispatch:

permissions:
  contents: read

jobs:
  build:
    name: Compilar Dark Bot
    runs-on: ubuntu-latest

    container:
      image: pspdev/pspdev:latest

    steps:
      - name: Baixar codigo do repositorio
        uses: actions/checkout@v4

      - name: Verificar diretorio
        run: |
          pwd
          ls -la
          find . -maxdepth 3 -type f

      - name: Preparar ambiente
        run: |
          command -v psp-config
          psp-config --pspsdk-path

      - name: Compilar Dark Bot
        run: |
          make clean
          make

      - name: Verificar EBOOT
        run: |
          ls -lh EBOOT.PBP

      - name: Publicar EBOOT.PBP
        uses: actions/upload-artifact@v4
        with:
          name: DarkBot-PSP
          path: EBOOT.PBP
          if-no-files-found: error
          retention-days: 14
