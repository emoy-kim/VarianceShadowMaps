#version 460

layout (location = 0) out vec4 final_moments;

void main()
{
   final_moments.r = gl_FragCoord.z;
   final_moments.g = gl_FragCoord.z * gl_FragCoord.z;

   //float dx = dFdx( gl_FragCoord.z );
   //float dy = dFdy( gl_FragCoord.z );
   //final_moments.g += 0.25f * (dx * dx + dy * dy);
}