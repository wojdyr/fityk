<?xml version="1.0"?>
<!-- XSLT stylesheet for generating command and option indices (DocBook -> DocBook) -->
<!-- (C) Marcin Wojdyr  -->
<!-- $Id$ -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="xml" indent="yes" 
          doctype-public="-//OASIS//DTD DocBook XML V4.2//EN" 
          doctype-system="http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd"/>

  <xsl:template match="/">
    <xsl:copy/>
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="*">
    <xsl:copy>
      <xsl:for-each select="@*">
        <xsl:copy/>
      </xsl:for-each>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="comment()|processing-instruction()">
    <xsl:copy/>
  </xsl:template>

  <xsl:variable name="command_def" select="//cmdsynopsis/command"/>
  <xsl:variable name="option_def" select="//parameter[@class='option'][local-name(..)!='link']"/>

  <!-- adding id for commands and options in text -->
  <xsl:template match="cmdsynopsis/command|parameter[@class='option']">
    <xsl:copy>
      <xsl:attribute name="id">
        <xsl:value-of select="generate-id()"/>
      </xsl:attribute>
      <xsl:for-each select="@*">
        <xsl:copy/>
      </xsl:for-each>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <!-- option index -->
  <xsl:template match="para[@role='optidx']">
    <simplelist>
      <xsl:for-each select="$option_def">
        <xsl:sort select="."/>
	<xsl:variable name="this_opt" select="$option_def[.=current()]"/>
        <xsl:if test="count(.|$this_opt[1]) = 1"> 
          <xsl:apply-templates select="." mode="o_idx"/>
	</xsl:if>
      </xsl:for-each>
    </simplelist>
  </xsl:template>

  <xsl:template match="*" mode="o_idx">
    <member>
      <xsl:element name="link">
        <xsl:attribute name="linkend">
          <xsl:value-of select="generate-id()"/>
        </xsl:attribute>
        <xsl:value-of select="."/> 
      </xsl:element>
      <xsl:variable name="cmd-try" select="../../para/cmdsynopsis/command"/>
      <xsl:choose>
       <xsl:when test="$cmd-try">
        (<xsl:value-of select="substring($cmd-try, 1, 1)"/>.set)
       </xsl:when>
       <xsl:otherwise>
        <xsl:variable name="cmd" 
			select="../../../section/para/cmdsynopsis/command"/>
        (<xsl:value-of select="substring($cmd, 1, 1)"/>.set)
       </xsl:otherwise>
      </xsl:choose>
    </member>
  </xsl:template>

  <!-- command index -->
  <xsl:template match="para[@role='cmdidx']">
    <simplelist>
      <xsl:for-each select="$command_def">
        <xsl:sort select="."/>
	<xsl:variable name="this_cmd" select="$command_def[.=current()]"/>
        <xsl:if test="count(.|$this_cmd[1]) = 1">
          <xsl:apply-templates select="." mode="c_idx"/>
	</xsl:if>
      </xsl:for-each>
    </simplelist>
  </xsl:template>

  <xsl:template match="*" mode="c_idx">
    <member>
      <xsl:element name="link">
        <xsl:attribute name="linkend">
          <xsl:value-of select="generate-id()"/>
        </xsl:attribute>
        <xsl:value-of select="."/> 
      </xsl:element>
    </member>
  </xsl:template>

</xsl:stylesheet>

