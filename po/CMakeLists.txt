set(LANGUAGES
  bg
  ca
  cs
  cy
  da_DK
  de
  el_GR
  es
  et
  fi
  fr
  he
  hi
  hu
  it
  ja
  ko
  lv
  nl
  pl
  pt
  pt_BR
  ro
  ru
  sk
  sv
  tr
  zh_CN
  zh_TW)

foreach(lang ${LANGUAGES})
  set(COMPILED_TRANSLATION "${lang}/swish.mo")
  install(FILES "${COMPILED_TRANSLATION}" DESTINATION "${lang}/LC_MESSAGES")
endforeach()
