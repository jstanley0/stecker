FROM ruby:2.3-alpine
RUN apt-get install g++
COPY ./stecker.rb /stecker.rb
RUN g++ stecker-engine.cpp -lpthread -o /stecker-engine
CMD ruby /stecker.rb
