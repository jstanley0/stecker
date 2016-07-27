FROM ruby:2.3-alpine
RUN apk add --no-cache g++
COPY ./stecker.rb /stecker.rb
COPY ./stecker-engine.cpp /stecker-engine.cpp
RUN g++ -O3 /stecker-engine.cpp -lpthread -o /stecker-engine
CMD ruby /stecker.rb
