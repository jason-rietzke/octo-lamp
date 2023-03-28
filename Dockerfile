FROM node:18.11.0-slim

WORKDIR /app

COPY /server/package*.json /app/
RUN cd /app && npm install
COPY /server /app
RUN cd /app && npm run build

EXPOSE 80
CMD cd /app/server && npm start
