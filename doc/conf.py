# -*- coding: utf-8 -*-

# Sphinx v1.0.4
#
# sphinx-build -d ./doctrees/ -b html . html

import sys, os
sys.path.append(os.path.abspath('.'))

extensions = ["sphinx.ext.pngmath", "sphinx.ext.extlinks", "fityk_ext"]

exclude_trees = ['html', 'latex', '.svn']
templates_path = ['.']
source_suffix = '.rst'
source_encoding = 'utf-8'
master_doc = 'fityk-manual'
project = 'Fityk'
copyright = '2001-2010, Fityk Developers'
version = '0.9.4'
release = version
default_role = None

#highlight_language = "none"
highlight_language = "fityk"
pygments_style = "trac"

html_theme = "sphinxdoc"
html_sidebars = {'index': [],
                 '**': ['globaltoc.html', 'sourcelink.html', 'searchbox.html']}
html_short_title = 'Fityk %s manual' % version
html_title = html_short_title
html_favicon = 'fityk.ico'
html_static_path = ['fityk-banner.png', 'fityk.css',
                    'unipress-button.png', 'wxwidgets_powered.png']
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
    'utf8extra': r"""
         \DeclareUnicodeCharacter{00A0}{\nobreakspace}
         \DeclareUnicodeCharacter{03C3}{\ensuremath{\sigma}}
         \DeclareUnicodeCharacter{2212}{\ensuremath{-}}
         """
}

#latex_appendices = ['appendix']
latex_show_pagerefs = True
latex_show_urls = True

sf_dir = 'http://downloads.sourceforge.net/fityk/'
msw_filename = 'fityk-%s-setup' % version
source_filename = 'fityk-%s' % version
extlinks = {
    'wiki': ('http://sourceforge.net/apps/trac/fityk/wiki/%s', ''),
    'sf-download-msw': (sf_dir + msw_filename + '%s', msw_filename),
    'sf-download-source': (sf_dir + source_filename + '%s', source_filename),
    }

