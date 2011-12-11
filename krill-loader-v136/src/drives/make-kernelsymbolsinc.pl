#!/usr/bin/perl

sub maybe_print {
    my $name = shift @_;
    my $address = shift @_;

    my @symbol = ($name, $address);
    push(@symbols, \@symbol);
}

$rw = open(FILE, shift ARGV) or die('failed to open input file');

while (defined($i = <FILE>)) {

    if ($i =~ /list:/) {
        $current_list = $i;
    }

    if ($current_list =~ 'Exports list:') {

        if ($i =~ /(\w+)\s+(\w+)\s+\w+\s+(\w+)\s+(\w+)/) {

            eval('$num1 = 0x' . $2 . '; $num2 = 0x' . $4);
            maybe_print($1, $num1);
            maybe_print($3, $num2);

        } elsif ($i =~ /(\w+)\s+(\w+)\s+\w+/) {

            eval('$num = 0x' . $2);
            maybe_print($1, $num);
        }
    }
}

my @sorted_symbols = sort { @$a[1] <=> @$b[1] } @symbols;

foreach my $symbol (@sorted_symbols) {
    printf "%-15s = \$%." . (@$symbol[1] < 256 ? '2' : '4') . "x\n", @$symbol[0], @$symbol[1];
}
