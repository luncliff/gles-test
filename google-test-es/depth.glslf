////////////////////////////////////////////////////////////////////////////////////////////////////////////
// depth-only fragment shader
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if GL_ES == 1

#ifdef GL_FRAGMENT_PRECISION_HIGH
	precision highp float;
#else
	precision mediump float;
#endif

#else
#endif

void main()
{
#if GL_ES == 1
	// nothing goes out to gl_FragColor or gl_FragData
#else
	gl_FragDepth = gl_FragCoord.z;
#endif
}
