#!/usr/bin/perl

sub maybe_print {
    my $name = shift @_;
    my $address = shift @_;

    if ($name !~ /fix|[A-Z]/) {
        if ((($address >= $diskio_start) && ($address <= $diskio_end))
         || (($address >= $diskio_install_start) && ($address <= $diskio_install_end))
         || (($address >= $diskio_zp_start) && ($address <= $diskio_zp_end))) {
            my @symbol = ($name, $address);
            push(@symbols, \@symbol);
        }
    }
}

$rw = open(FILE, shift ARGV);

while (defined($i = <FILE>)) {

    if ($i =~ /list:/) {
        $current_list = $i;
    }

    if ($current_list =~ 'Segment list:') {

        if ($i =~ /DISKIO\w*\s+(\w+)\s+(\w+)/) {
            eval('$_start = 0x' . $1 . '; $_end = 0x' . $2);
        }

        if ($i =~ /DISKIO_ZP/) {
            $diskio_zp_start = $_start;
            $diskio_zp_end = $_end;
        } elsif ($i =~ /DISKIO_INSTALL/) {
            $diskio_install_start = $_start;
            $diskio_install_end = $_end;
        } elsif ($i =~ /DISKIO/) {
            $diskio_start = $_start;
            $diskio_end = $_end;
        }
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

my @oldsymbol;
print "; zeropage\n";

foreach my $symbol (@sorted_symbols) {
    if ((@oldsymbol[1] < $diskio_install_start) && (@$symbol[1] >= $diskio_install_start)) {
        print "\n; install\n";
    }
    if ((@oldsymbol[1] < $diskio_start) && (@$symbol[1] >= $diskio_start)) {
        print "\n; resident\n";
    }

    printf "%-15s = \$%." . (@$symbol[1] < 256 ? '2' : '4') . "x\n", @$symbol[0], @$symbol[1];

    @oldsymbol = @$symbol;
}
