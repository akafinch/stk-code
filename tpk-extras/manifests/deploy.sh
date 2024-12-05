echo "Deploying STK deployment and service manfests."
kubectl --kubeconfig=kc-develop apply -f deployment.yaml -f service.yaml
