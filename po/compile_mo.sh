for lang in *; do
   if [ -d $lang ]; then
      msgfmt -c --statistics --verbose -o $lang/swish.mo $lang/swish.po
   fi
done
