#   …   or create a new repository on the command line


echo "# the third commit is FTP.c client" >> README.md
git init
git add .
#git commit -m "second commit---Openwrt SDK"
git commit -m "third commit---FTP.c client"
git remote add origin https://github.com/jeanslee/testgit.git
git push -u origin master
git push
