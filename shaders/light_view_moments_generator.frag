#version 460

layout (location = 0) out vec2 final_moments;

void main()
{
   final_moments.r = gl_FragDepth;
   final_moments.g = gl_FragDepth * gl_FragDepth;

   //float dx = dFdx( gl_FragDepth );
   //float dy = dFdy( gl_FragDepth );
   //final_moments.g += 0.25f * (dx * dx + dy * dy);
}