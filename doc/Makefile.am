man1_MANS = tglng.1
nobase_doc_DATA = examples/emacs_tglng.el

tglng.1: ../README
	a2x -L -D . --doctype manpage --format manpage ../README || true
	touch tglng.1
	a2x -L -D . --doctype manpage --format pdf ../README || true

