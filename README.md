# redirectr.ee

`redirectr.ee` es un servidor escrito en C que utiliza subdominios personalizados para redirigir a enlaces específicos,
similar a servicios como [linktr.ee](https://linktr.ee). Este servidor te permite gestionar múltiples enlaces utilizando
diferentes subdominios y redirigir a los usuarios a tus plataformas preferidas.

## Compilación

Compila el servidor con el siguiente comando en Linux o macOS:

```bash
make clean && make
```

## Ejecución

Una vez compilado, puedes ejecutar el servidor con el siguiente comando, proporcionando el archivo de configuración config.ini:

```bash
./build/redirectr < config.ini
```

El servidor leerá la configuración de los subdominios y redirigirá a los usuarios a las URL especificadas en el archivo
de configuración.

## Configuración

```ini
# Server
host=0.0.0.0
port=8081

# Subdominios
twitter.albo.ar=https://twitter.com/4lb0
x.albo.ar=https://twitter.com/4lb0
github.albo.ar=https://github.com/4lb0
ig.albo.ar=https://www.instagram.com/albo.ar
instagram.albo.ar=https://www.instagram.com/albo.ar

# Default
*=https://albo.ar
```
