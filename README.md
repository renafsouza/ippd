# Algoritmo de compressão de vídeos YUV utilizando openMP

Esse repositório é a implementação de um trabalho de Introdução ao Processamento Paralelo e Distribuido.

Consiste de um algoritmo que comprime e descomprime arquivos de video do formato YUV.

## Instalação

Utilize o seguinte comando para compilar o programa

```bash
make
```

## Uso

Listar menu de ajuda:
```bash
compress -h
decompress -h
```
Comprimir um arquivo:
```bash
compress file.yuv 120 640 360
```

Descomprimir um arquivo:
```bash
decompress file.yuv.cmp 120 640 360
```
