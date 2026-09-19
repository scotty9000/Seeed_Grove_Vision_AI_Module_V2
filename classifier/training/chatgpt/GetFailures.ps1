

$project = "1116365"
$key = "ei_6bd2a022a41c762fb90c9a3f75aa7f76c1038207925a51c4"

Invoke-RestMethod `
  -Uri "https://studio.edgeimpulse.com/v1/api/$project/classify/all/result" `
  -Headers @{ "x-api-key" = $key } |
  ConvertTo-Json -Depth 20 |
  Out-File model_test_results.json
