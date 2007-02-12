<?xml version="1.0"?>
<!-- $Id$ -->

<!-- a driver stylesheet for htmlhelp output (used with xmlto -m option) -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:param name="html.stylesheet" select="'html.css'"/>

  <!-- no entry for root element in ToC - expanded ToC as a default -->
  <xsl:param name="htmlhelp.hhc.show.root" select="0" />

  <!-- Jump1 button -->
  <xsl:param name="htmlhelp.button.jump1" select="1" />
  <xsl:param name="htmlhelp.button.jump1.url" 
	                 select="'http://fityk.sourceforge.net'" />
  <xsl:param name="htmlhelp.button.jump1.title" select="'Fityk HomePage'" />

</xsl:stylesheet>

