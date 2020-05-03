### Part 4 - Our First Tests (and Bugs)
Here we use `rspec` to do tests, thus we need to install **ruby**  

- `sudo apt install ruby-full`
- `sudo gem install rspec bundler`
- `mkdir spec && touch Gemfile && touch spec/main_spec.rb`

For `Gemfile`, its content is  
```
source 'https://rubygems.org'
gem "rspec"
```  
After finishing writing tests in file `main_spec.rb`,just `bundle exec rspec`