set LOCATION=antlr-4.9.2-complete.jar
java -jar %LOCATION% -Dlanguage=Cpp -listener -visitor -o generated/ -package antlrcpptest Mu.g4 