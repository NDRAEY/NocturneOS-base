default:
    @just --list

[working-directory: 'rust']
update:
    cargo update --recursive

clean_tree:
	git clean -dfX

[working-directory: 'rust']
test: 
    cargo test -p noct-path
