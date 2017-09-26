#!/usr/bin/perl

##  Author:   Yan Tian <tdhlshx@yahoo.com.cn>
##  Date:     Nov. 22, 2005
##  Function: Use this scrpit to delete ftp user of this ftp server.

use Getopt::Std;
use POSIX;

require './ftp_usrs_mgnt.ph';

my $usrname;
my @usr_tmp;

sub show_usage()
{
	print "Usage: $0	-u username\n";
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
	getopts('u:', \%opts);
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
}

sub chk_usr_name()
{
	my $chk = "";
	if(-f $userinfo)
	{
		$chk=`cat $userinfo | grep ^$usrname:`;
	}
	if("$chk" eq "")
	{
		error_exit("User $usrname does not exist!");
	}
}

get_opts();
chk_usr_name();
@usr_tmp = `cat $userinfo | grep -v ^$usrname:`;
print @usr_tmp;

open(USR_INFO, "+> $userinfo") or die ("Can't open file $userinfo\n");
print USR_INFO @usr_tmp;
close(USR_INFO);
