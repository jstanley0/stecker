FROM ruby:2.3-alpine
RUN apk add --no-cache g++
COPY ./make.sh /make.sh
COPY ./stecker.rb /stecker.rb
COPY ./stecker-engine.cpp /stecker-engine.cpp
RUN /make.sh
CMD ruby /stecker.rb
