#!/usr/bin/perl -w

use Fityk;

my $ftk = new Fityk::Fityk;

printf "libfityk version: %s\n", $ftk->get_info("version", 0);
printf "ln(2) = %g\n", $ftk->calculate_expr("ln(2)");

# check error handling
print "Croaks on error: ", ($ftk->get_throws() ? "yes" : "no" ), "\n";
$ftk->set_throws(0);
print "Croaks on error: ", ($ftk->get_throws() ? "yes" : "no" ), "\n";
$ftk->last_error() and print "last error: ", $ftk->last_error(), "\n";
$ftk->execute("bleh"); # SyntaxError
$ftk->last_error() and print "last error: ", $ftk->last_error(), "\n";
$ftk->execute("fit");  # ExecuteError
$ftk->last_error() and print "last error: ", $ftk->last_error(), "\n";
$ftk->clear_last_error();

$ftk->redir_messages(STDOUT); # redirect fityk messages to stdout
#$ftk->redir_messages(0); # this disables fityk messages

my $filename = "nacl01.dat";
print "Reading data from $filename ...\n";
$ftk->execute("\@0 < '$filename'");
printf "Data info:\n%s\n", $ftk->get_info('data', 0);

$ftk->execute('guess %gauss = Gaussian');
print "Fitting...\n";
$ftk->execute("fit");
printf "WSSR=%f\n", $ftk->get_wssr();
printf "Gaussian center: %g\n", $ftk->calculate_expr("%gauss.center");

$ftk->execute("info state >tmp_save.fit");

$ftk->DESTROY();
