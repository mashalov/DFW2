grammar Equations;

input 
    : variables? main init? NEWLINE* EOF
    ;

main : MAIN equationsys;
init : INIT equationsys;
variables: VARS vardefines;

vardefines
    : LCB vardefineline* NEWLINE* RCB
    ;

equationsys
    : LCB equationline* NEWLINE* RCB
    ;

varlist
    : VARIABLE (COMMA VARIABLE)*
    ;

vardefineline 
    : (NEWLINE)+ vardefine
    ;
 
vardefine
    : constvardefine
    | extvardefine
    ;
    
constvardefine
    : CONST varlist
    ;

extvardefine
    : EXTERNAL varlist
    ;

equation
    :  left = expression EQUAL right = expression
    ;

equationline
    : (NEWLINE)+ equation
    ;

expressionlist
    : expression (COMMA expression)*
    ;

intlist
    : INTEGER (COMMA INTEGER)*
    ;

constvalue
    : FLOAT
    | INTEGER
    ;

modlink
    :  VARIABLE LSB intlist RSB DOT VARIABLE
    ;

modlinkbase
    : BAR modlink
    ;

expression 
    : constvalue                                                #realconst  
    | VARIABLE                                                  #variable
    | modlinkbase                                               #modellinkbase
    | modlink                                                   #modellink
    | LB expression RB                                          #braces
    | <assoc=right> expression POW expression                   #pow
    | <assoc=right> op = (PLUS|MINUS) expression                #unary
    | left = expression op = (MUL|DIV) right = expression       #infix
    | left = expression op = (PLUS|MINUS) right = expression    #infix
    | left = expression op = (HIGH|LOW) right = expression      #highlow
    | left = expression op = (OR|AND) right = expression        #andor
    | func = VARIABLE LB expressionlist? RB                     #function
    ;

FLOAT
    :   DOT DIGITS (Exponent)?
    |   DIGITS DOT Exponent
    |   DIGITS (DOT (DIGITS (Exponent)?)? | Exponent)
    ;

INTEGER : DIGITS;

fragment DIGITS : [0-9]+;
fragment Exponent
    :    ('e' | 'E') ( '+' | '-' )? DIGITS
    ;

MAIN : NEWLINE* 'main' NEWLINE* ;
INIT : NEWLINE* 'init' NEWLINE* ;
VARS : NEWLINE* 'variables' NEWLINE* ;

CONST: 'const';
EXTERNAL : 'external';
VARIABLE     : [a-zA-Z_][a-zA-Z_0-9]*;
NEWLINE:'\r'? '\n';
LINE_COMMENT : '//' ~[\r\n]* -> skip;
EQUAL : '=';
BAR: '#';
AND: '&';
OR:  '|';
LB: '(';
RB: ')';
LCB: '{';
RCB: '}';
LSB : '[';
RSB : ']';
DOT : '.';
COMMA: ',';
PLUS: '+';
MINUS: '-';
MUL: '*';
DIV: '/';
POW: '^';
HIGH : '>';
LOW  : '<';
WS : [ \t]+ -> skip;
