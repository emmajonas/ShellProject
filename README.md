# Shell Project

Designed and implemented a simple shell in C for my Operating Systems class.

## Supports

- all basic shell commands
- `cd` command to change directory
- pipes with `|`
- input and output redirection with `>` or `<`
- background processes by adding `&` to the end of any command (still buggy with interactive processes, like `top &`)
- `!!` history feature to execute the previous command
- exit command

### Note

- no support for quoting arguments or escaping whitespace
- no support for the chaining of pipes and/or redirection operators (only accepts one per input line)
- no job control
- commands must be in one line
- max input length is 80 characters
- arguments must be separated by whitespace (including `&`)
