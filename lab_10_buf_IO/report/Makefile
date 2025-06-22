FILE:= report

.PHONY : report
report:
	pdflatex $(FILE).tex
	bibtex $(FILE).aux
	pdflatex $(FILE).tex
	pdflatex $(FILE).tex
	make clean

.PHONY : clean
clean :
	rm *.aux *.bbl *.blg *.log *.out *.toc *.fls *.gz *.fdb_latexmk
