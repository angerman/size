macho-info: macho-info.c
	cc macho-info.c -o macho-info

test: macho-info
	./macho-info Main

Main: Main.hs
	ghc Main.hs

.PHONY: clean
clean:
	rm -f macho-info