:: File viegophr.bat
::
:: generate viegophr.tex and viegophr.product files from the
:: hypertext source
::
:: 1992-08-29
:: ---------------------------------------------------------------------------
::
:: 1. generate the doucmentation: viegophr.tex
hyxseq -fseq.doc -O$.s viegophr.hyx
chksgml -Oviegophr.tex $.s
translit -tviegophr.t1 viegophr.tex
:: goto END
::
:: 2. generate the product file: viegophr.pro
hyxseq -fseq.pro -O$.s viegophr.hyx
chksgml -O$.s2 $.s
awk " {print}" $.s2 >viegophr.pro
::
:: 3. generate the LaTeX file for the product printout
awk rosetex.awk viegophr.pro >vieg-pro.tex
translit -tviegophr.t1 vieg-pro.tex
::
:: 4. generate the mini doucmentation: vieg-md.tex
hyxseq -fseq.mini -O$.s viegophr.hyx
chksgml -Ovieg-md.tex $.s
translit -tviegophr.t1 vieg-md.tex
::
:: 5. generate the mini product file: vieg-mp.pro
hyxseq -fseq.mini.pro -O$.s viegophr.hyx
chksgml -O$.s2 $.s
awk " {print}" $.s2 >vieg-mp.pro
::
:: 6. generate the LaTeX file for the product printout
awk rosetex.awk vieg-mp.pro >vieg-mp.tex
translit -tviegophr.t1 vieg-mp.tex
:COPY
rem copy viegophr.tex l:
:END
