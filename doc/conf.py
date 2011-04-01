# -*- coding: utf-8 -*-

# Sphinx v1.0.7
#
# sphinx-build -d ./doctrees/ -b html . html

import sys, os
sys.path.append(os.path.abspath('.'))

extensions = ["sphinx.ext.pngmath", "sphinx.ext.extlinks", "fityk_ext"]

exclude_trees = ['html', 'latex', '.svn']
exclude_patterns = ['index.rst', 'screens.rst']
templates_path = ['.']
source_suffix = '.rst'
source_encoding = 'utf-8'
master_doc = 'fityk-manual'
project = 'Fityk'
copyright = '2001-2011, Fityk Developers'
version = '1.0.1'
release = version
default_role = None

#highlight_language = "none"
highlight_language = "fityk"
pygments_style = "trac"

html_theme = "sphinxdoc"
html_sidebars = {'index': [],
                 'screens': [],
                 '**': ['globaltoc.html', 'sourcelink.html', 'searchbox.html']}
html_title = 'Fityk %s manual' % version
html_short_title = 'Manual'
html_favicon = 'fityk.ico'
html_static_path = ['fityk-banner.png', 'fityk.css']
html_style = 'fityk.css'
html_last_updated_fmt = '%Y-%m-%d'
html_use_smartypants = True
html_use_modindex = False
html_use_index = False
html_add_permalinks = False
#html_compact_lists = True
html_show_copyright = False

latex_documents = [
  ('fityk-manual', 'fityk-manual.tex', 'Fityk manual', '', 'manual', True),
]
latex_logo = 'fityk-banner.pdf'
latex_elements = {
    'papersize': 'a4paper', # 'letterpaper'
    'pointsize': '10pt,oneside,openany',
    #'classoptions': ',oneside,openany',
    'inputenc': r"""
         \usepackage{ucs}
         \usepackage[utf8x]{inputenc}""",
    'utf8extra': '',

    # put notes into boxes
    'preamble': r"""
\usepackage{ifthen}
\definecolor{gray03}{gray}{0.3}
\let\origbeginnotice\notice
\let\origendnotice\endnotice

\renewenvironment{notice}[2]{%
  \def\noticetype{#1}
  \def\inTheGui{\equal{#2}{In the GUI}}
  \ifthenelse{\equal{#1}{note}}{%
    \setlength{\fboxrule}{1pt}
    \setlength{\fboxsep}{6pt}
    \setlength{\mylen}{\linewidth}
    \addtolength{\mylen}{-2\fboxsep}
    \addtolength{\mylen}{-4\fboxrule}
    %\setlength{\shadowsize}{3pt}
    \Sbox
    \minipage{\mylen}
    \ifthenelse{\inTheGui}{\color{gray03}}{}
    \par\strong{#2}
  }{%
  \origbeginnotice{#1}{#2}%
  }
}{%
  \ifthenelse{\equal{\noticetype}{note}}{%
    \endminipage
    \endSbox
    \ifthenelse{\inTheGui}{\ovalbox{\TheSbox}}{\fbox{\TheSbox}}
  }{%
  \origendnotice%
  }
}
"""
}

latex_show_pagerefs = True
latex_show_urls = True

# determine vertical alignment of the math PNGs
pngmath_use_preview = True

dl_dir = 'http://fityk.nieto.pl/subscribers/'
dl_prefix = 'fityk-%s' % version
extlinks = {
    'wiki': ('https://github.com/wojdyr/fityk/wiki/%s', ''),
    'download': (dl_dir + dl_prefix + '%s', dl_prefix),
    }

