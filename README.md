# octo-lamp

## Server

Run server as `docker container` and make sure to set the `PORT` and `SECRET` environment variables.

The `SECRET` is used to authenticate the requests from the Arduino.
Set the same `SECRET` in the Arduino sketch as `SERVER_SECRET` variable alongside the `SERVER_URL`.
