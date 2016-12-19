# -*- coding: utf-8 -*-

needs_sphinx = '1.5'

import sys, os
sys.path.append(os.path.abspath('.'))

extensions = ["sphinx.ext.imgmath", "sphinx.ext.extlinks", "fityk_ext"]

exclude_patterns = ['html', 'latex']
if not os.getenv('BUILD_WEBSITE'):
    exclude_patterns += ['index.rst', 'screens.rst']

if os.getenv('READTHEDOCS'):
    # RTD works better with index.html
    html_additional_pages = { 'index': '_build/html/fityk-manual.html' }

html_theme_path = ['.']
source_suffix = '.rst'
source_encoding = 'utf-8'
master_doc = 'fityk-manual'
project = 'Fityk'
version = '1.3.1'
release = version
default_role = None
suppress_warnings = ["ref.option"]

#highlight_language = "none"
highlight_language = "fityk"
pygments_style = "trac"

#html_theme = "sphinxdoc"
html_theme = 'fitheme'

html_theme_options = {
    'github_blob': 'wojdyr/fityk/blob/master/doc',
    'logo': 'fityk-logo.png',
    'logo_caption': 'curve fitting,&nbsp; data analysis',
    'website': 'http://fityk.nieto.pl/',
    'wiki': 'https://github.com/wojdyr/fityk/wiki',
}
if os.getenv('BUILD_WEBSITE'):
    html_theme_options['analytics_id'] = 'UA-20519098-2'
html_sidebars = { 'index': [], 'screens': [], '**': ['side.html'] }
html_title = 'Fityk %s manual' % version
html_short_title = 'Manual'
html_favicon = 'fityk.ico'
html_static_path = ['fityk-logo.png', 'img/mouse16.png']
html_last_updated_fmt = '%Y-%m-%d'
html_use_smartypants = True
html_use_modindex = False
html_use_index = False
#html_add_permalinks = False
#html_compact_lists = True
html_show_copyright = False
html_copy_source = False  # we link directly to GitHub

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
""",
  'printindex' : ''
}

#latex_show_pagerefs = True
latex_show_urls = 'footnote'

# determine vertical alignment of the math PNGs
imgmath_use_preview = True

dl_dir = 'http://fityk.nieto.pl/subscribers/'
dl_prefix = 'fityk-%s' % version
extlinks = {
    'wiki': ('https://github.com/wojdyr/fityk/wiki/%s', ''),
    'subscribers_download': (dl_dir + dl_prefix + '%s', dl_prefix),
    }

