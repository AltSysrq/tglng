SUBDIRS = src
doc/tglng.1: README
	a2x -L -D doc --doctype manpage --format manpage README

# Depend on tglng.1 to prevent them from building in parallel
# (a2x uses predictable filenames in the CWD and will collide with itself).
doc/README.pdf: README doc/tglng.1
	a2x -L -D doc --doctype manpage --format pdf README

docs: doc/tglng.1 doc/README.pdf
