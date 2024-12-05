echo "---DEPLOYMENTS---"
kubectl --kubeconfig=kc-develop get deployments
echo "---SERVICES------"
kubectl --kubeconfig=kc-develop get services
echo "---PODS----------"
kubectl --kubeconfig=kc-develop get pods
