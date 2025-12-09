FROM node:24 AS compiler
WORKDIR /compiler

COPY . .

RUN npm install && npm run build:docker

FROM nginx
WORKDIR /website

COPY --from=compiler /compiler/dist ./