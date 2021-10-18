
import re

from pygments.lexer import RegexLexer
from pygments.token import Text, Name, Comment, String, Generic
from sphinx import addnodes
from docutils import nodes

class FitykLexer(RegexLexer):
    name = 'fityklexer'

    tokens = {
            'root': [
                (r"'[^']*'", String.Single),
                (r'[#].*?$', Comment),
                (r'^=-> ', Generic.Prompt),
                (r"[^'#\n]+", Text),
                ],
            }

comment_re = re.compile(r'(\(\*.*?\*\))')

def doctree_read(app, doctree):
    env = app.builder.env
    for node in doctree.traverse(addnodes.productionlist):
        for production in node:
            if not isinstance(production, addnodes.production):
                continue
            if not isinstance(production[-1], nodes.Text):
                continue
            parts = comment_re.split(production.pop().astext())
            new_nodes = []
            for s in parts:
                if comment_re.match(s):
                    new_nodes.append(nodes.emphasis(s, s))
                elif s:
                    new_nodes.append(nodes.Text(s))
            production += new_nodes

def role_ftype(name, rawtext, text, lineno, inliner, options={}, content=[]):
    node = nodes.strong(text, text)
    assert text.count('(') in (0, 1)
    assert text.count('(') == text.count(')')
    assert ')' not in text or text.endswith(')')
    m = re.search(r'\((.*?)\)', text)
    node['ids'] = [m.group(1) if m else text]
    return [node], []

def setup(app):
    app.add_lexer('fityk', FitykLexer);
    app.connect('doctree-read', doctree_read)
    app.add_role('ftype', role_ftype)

