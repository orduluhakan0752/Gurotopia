FROM ubuntu:latest

RUN apt update && apt install -y build-essential libssl-dev libmariadb-dev

COPY . .

RUN ls -lah /items.dat
RUN make -j$(nproc)

CMD ["/main.out"]
