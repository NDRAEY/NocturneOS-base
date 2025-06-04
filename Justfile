update:
 	cd rust && cargo update --recursive && cd ..;

clean_tree:
	git clean -dfX
