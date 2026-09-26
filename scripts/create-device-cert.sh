#!/usr/bin/env bash
# Create the device's X.509 certificate + keys with AWS IoT, and download the
# Amazon root CA. Everything lands in certs/ (git-ignored).
#
# Done outside CDK on purpose: CloudFormation can't return a private key, and
# the key should only ever exist on this machine.
#
# Usage:  scripts/create-device-cert.sh
# Then:   put the printed ARN into infra/cdk.json -> "certificateArn", and deploy.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CERTS="$ROOT/certs"

if [[ -f "$CERTS/device.pem.crt" ]]; then
  echo "certs/device.pem.crt already exists — not overwriting." >&2
  echo "Delete certs/ (and deactivate the old cert in AWS) to start again." >&2
  exit 1
fi

mkdir -p "$CERTS"

CERT_ARN=$(aws iot create-keys-and-certificate \
  --set-as-active \
  --certificate-pem-outfile "$CERTS/device.pem.crt" \
  --public-key-outfile "$CERTS/public.pem.key" \
  --private-key-outfile "$CERTS/private.pem.key" \
  --query certificateArn --output text)

echo "$CERT_ARN" > "$CERTS/certificate-arn.txt"

curl -sSf -o "$CERTS/AmazonRootCA1.pem" https://www.amazontrust.com/repository/AmazonRootCA1.pem

echo
echo "Certificate created:"
echo "  $CERT_ARN"
echo
echo "Next: set \"certificateArn\" in infra/cdk.json to that ARN, then 'cdk deploy ClockScaleStack'."
