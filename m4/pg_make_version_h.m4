#
# create version header
#
# PG_MAKE_VERSION_H([header])

AC_DEFUN([PG_MAKE_VERSION_H],
[

rm -f $1

printf "/******************************************************************************/\n" >> $1
printf "/* version.h                                                                  */\n" >> $1
printf "/******************************************************************************/\n" >> $1
printf "/* this file is automaticaly generated                                        */\n" >> $1
printf "/******************************************************************************/\n" >> $1
printf "\n" >> $1
printf "#ifndef __%s_VERSION_H__\n" ${PACKAGE_NAME} >> $1
printf "#define __%s_VERSION_H__\n" ${PACKAGE_NAME} >> $1
printf "\n" >> $1
printf "#define %s_VERSION \"%s\"\n" ${PACKAGE_NAME} ${PACKAGE_VERSION} >> $1
printf "\n" >> $1
printf "#endif //__%s_VERSION_H__\n" ${PACKAGE_NAME} >> $1
printf "\n" >> $1
printf "/******************************************************************************/\n" >> $1
printf "/* end of version.h                                                           */\n" >> $1
printf "/******************************************************************************/\n" >> $1

])
