#!/usr/bin/env bash

lang=${1}
heat="${WIX}bin\heat.exe"

# strip underscores and make uppercase for WiX component name
componentprefix="$(echo ${lang} | tr -d _ | tr '[a-z]' '[A-Z]')"

# Template directory structure
mkdir ${lang}
mkdir ${lang}/LC_MESSAGES
touch ${lang}/LC_MESSAGES/swish.mo

"${heat}" dir ${lang} -cg ${componentprefix}Trans -dr INSTALLDIR -var var.SolutionDir -ag -indent 2 -o ${lang}.wxs

rm ${lang}/LC_MESSAGES/swish.mo
rmdir ${lang}/LC_MESSAGES
rmdir ${lang}

# The source tree doesn't match the template directory structure we
# built the WXS file from, above.  So we fix the file to match the
# source
sed -i 's/(var.SolutionDir)\\LC_MESSAGES/(var.SolutionDir)\\po\\'"${lang}"'/g' ${lang}.wxs


# The new fragment must appear in the WiX file to be included in the installer
insertionmarker='<ComponentGroup Id="Translations">'
sed -i "s/${insertionmarker}/${insertionmarker}\\n      <ComponentGroupRef Id=\"${componentprefix}Trans\" \\/>/g" swish.wxs

# And must be in the VS project or it won't get compiled
insertionmarker='<Compile Include="swish.wxs" \/>'
sed -i "s/${insertionmarker}/${insertionmarker}\\n    <Compile Include=\"${lang}.wxs\" \\/>/g" wix.wixproj
