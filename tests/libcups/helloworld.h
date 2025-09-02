#include <stdio.h>
#include <mbedtls/x509.h>
#include <cups/cups.h>
#include <zephyr/net/dns_sd.h>

int main()
{
    printf("Hello libcups %f! %d %d\n", CUPS_VERSION, MBEDTLS_X509_MAX_DN_NAME_SIZE, DNS_SD_DOMAIN_MAX_SIZE);
    return 0;
}
