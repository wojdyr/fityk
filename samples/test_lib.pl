#!/usr/bin/perl -w

require fityk;

$ftk = new fityk::Fityk;

printf "libfityk version: %s\n", $ftk->get_info("version", 0);
printf "ln(2) = %g\n", $ftk->get_info("ln(2)");

# check error handling
#$ftk->execute("bleh"); # SyntaxError
#$ftk->execute("fit");  # ExecuteError

$ftk->redir_messages(STDOUT); # redirect fityk messages to stdout
#$ftk->redir_messages(0); # this disables fityk messages

$filename = "nacl01.dat";
print "Reading data from $filename ...\n";
$ftk->execute("\@0 < '$filename'");
printf "Data info:\n%s\n", $ftk->get_info('data in @0');

$ftk->execute('%gauss = guess Gaussian');
print "Fitting...\n";
$ftk->execute("fit");
printf "WSSR=%f\n", $ftk->get_wssr();
printf "Gaussian center: %g\n", $ftk->get_variable_value("%gauss.center");

$ftk->execute("dump >tmp_dump.fit");

$ftk->DESTROY();
