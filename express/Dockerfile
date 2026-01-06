FROM node:24
WORKDIR /api

RUN npm install tsx -g
RUN npm install @types/express express

COPY ./express/server.ts ./server.ts

EXPOSE 3000

CMD ["tsx","server.ts"]