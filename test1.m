this is the first line
{{#comment}}

{{{{}} }}

{{a.b}}

{{if a.b}}
a.b true
{{else}}
a.b false (else)
{{endif}}

{{for v in a.b}}
{{v.key}} {{v.value}}
{{endfor}}

{{for v in a.b}}
{{if a.b}}
{{for v in a.b}}
a.b true
{{endfor}}
{{else}}
{{for v in a.b}}
a.b false (else)
{{endfor}}
{{if c}}
blah
{{endif}}
{{endif}}
{{endfor}}


this is the final line