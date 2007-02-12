<?xml version="1.0"?> 
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:import href="/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl"/> 
	<xsl:param name="paper.type" select="'A4'"/> 

	<xsl:attribute-set name="monospace.verbatim.properties"
	                 use-attribute-sets="verbatim.properties">
	  <xsl:attribute name="font-family">
	    <xsl:value-of select="$monospace.font.family"/>
	  </xsl:attribute>
	  <xsl:attribute name="font-size">
	    <xsl:value-of select="$body.font.master * 0.8"/>
	    <xsl:text>pt</xsl:text>
	  </xsl:attribute>
	  <xsl:attribute name="border-color">#000000</xsl:attribute>
	  <xsl:attribute name="border-style">solid</xsl:attribute>
	  <xsl:attribute name="border-width">thin</xsl:attribute>
	</xsl:attribute-set>

	<xsl:param name="admon.textlabel" select="0"></xsl:param>
	<xsl:param name="admon.graphics" select="0"></xsl:param>

	<xsl:attribute-set name="component.title.properties">
	  <xsl:attribute name="font-size">
	    <xsl:value-of select="$body.font.master * 1.9"></xsl:value-of>
	    <xsl:text>pt</xsl:text>
	  </xsl:attribute>
	</xsl:attribute-set>

	<xsl:attribute-set name="section.title.level1.properties">
	  <xsl:attribute name="font-size">
	    <xsl:value-of select="$body.font.master * 1.5"></xsl:value-of>
	    <xsl:text>pt</xsl:text>
	  </xsl:attribute>
	</xsl:attribute-set>

	<xsl:attribute-set name="section.title.level2.properties">
	  <xsl:attribute name="font-size">
	    <xsl:value-of select="$body.font.master * 1.3"></xsl:value-of>
	    <xsl:text>pt</xsl:text>
	  </xsl:attribute>
	</xsl:attribute-set>

	<xsl:attribute-set name="section.title.level3.properties">
	  <xsl:attribute name="font-size">
	    <xsl:value-of select="$body.font.master * 1.15"></xsl:value-of>
	    <xsl:text>pt</xsl:text>
	  </xsl:attribute>
	</xsl:attribute-set>

	<xsl:param name="headers.on.blank.pages" select="0"></xsl:param>

</xsl:stylesheet>
