FROM ruby:2.3-alpine
COPY ./stecker.rb /stecker.rb
CMD ruby /stecker.rb
