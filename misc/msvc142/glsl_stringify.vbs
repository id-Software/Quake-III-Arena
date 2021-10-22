Dim fso, infile, outfile, line
Set fso = CreateObject("Scripting.FileSystemObject")
Set infile = fso.OpenTextFile(WScript.Arguments(0))
Set outfile = fso.CreateTextFile(WScript.Arguments(1), True)

outfile.WriteLine("const char *fallbackShader_" & fso.GetBaseName(WScript.Arguments(0)) & " =")
While Not infile.AtEndOfStream
	line = infile.ReadLine
	line = Replace(line, "\", "\\")
	line = Replace(line, Chr(9), "\t")
	line = Replace(line, Chr(34), "\" & chr(34))
	line = Chr(34) & line & "\n" & Chr(34)
	outfile.WriteLine(line)
Wend
outfile.WriteLine(";")

infile.Close
outfile.Close