idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")
# Cria uma imagem SPIFFS a partir do conteúdo do diretório 'spiffs_image'
# que se encaixa na partição chamada 'storage'. FLASH_IN_PROJECT indica que
# a imagem gerada deve ser flashed quando todo o projeto é flashado para
# o destino com 'idf.py -p PORT flash'.                   
