#!/usr/bin/perl

##  Author:   Yan Tian <tdhlshx@yahoo.com.cn>
##  Date:     Nov. 17, 2005
##  Function: Use this scrpit to add ftp users of this ftp server.

use Getopt::Std;
use POSIX;

require './ftp_usrs_mgnt.ph';

my $usrname, $passwd, $homedir, $level;

sub show_usage() 
{
	print "Usage: $0 	-u username -p passwd\n";
	print "			-d <home directory> -l level\n";
	print "level is a number from 0 to 3\n";
	print "level 0 -- list\n";
	print "level 1 -- list & download\n";
	print "level 2 -- list & upload\n";
	print "level 3 -- list & download & upload\n";
	exit 0;
}

sub error_exit($) 
{
	print "$_[0]\n";
	exit 1;
}

sub get_opts()
{

	my %opts;
	getopts('u:p:d:l:', \%opts);

	my $count_opt = keys %opts;
	if($count_opt == 0) 
	{
		show_usage();
	}
	if(not defined($opts{'u'})) 
	{
		print "You must assign a username with -u\n";
		show_usage();
	}
	else 
	{
		$usrname=$opts{'u'};
	}
	if(not defined($opts{'p'})) 
	{
		print "You must assign a passwd with -p\n";
		show_usage();
	}
	else 
	{
		$passwd=$opts{'p'};
	}
	if(not defined($opts{'d'}))
	{
		$homedir="/home";
	}
	else 
	{
		$homedir=$opts{'d'};
	}
	if(not defined($opts{'l'}))
	{
		$level = 0;
	}
	else 
	{
		$level = $opts{'l'};
	}
}

sub chk_user_name()
{
	my $chk_user = "";
	if(-f $userinfo)
	{
		$chk_user=`cat $userinfo | grep ^$usrname:`;
	}
	if("$chk_user" ne "")
	{
		error_exit("User $usrname exists!");
	}
}

sub check_opts() 
{
	if($usrname !~ /^[0-9a-zA-Z_]+$/) 
	{
		error_exit("Invalid usrname!");
	}
	if(! -d $homedir)
	{
		error_exit("Invalid home directory!");
	}
	if(($level < 0) || ($level > 3))
	{
		error_exit("Invalid level!");
	}
}

sub generate_salt()
{
	my @array=('.', '/', 0..9, 'A'..'Z', 'a'..'z');
	my $salt="\$1\$";
	for(my $i=0;$i<8;$i++)
	{
		my $m=int(rand(64));
		$salt.=$array[$m];
	}
	$salt.="\$";
	return $salt;
}


sub my_crypt($)
{
	my $salt = generate_salt();
	return crypt("$_[0]", "$salt");
}

#main process for add user.
get_opts();
chk_user_name();
check_opts();

my $crypt_pass = my_crypt("$passwd");
my $new_item = "$usrname:$crypt_pass:$homedir:$level";
open(USR_INFO,">>$userinfo") or die("Can't open file $userinfo\n");
print USR_INFO $new_item,"\n";
close(USR_INFO);

