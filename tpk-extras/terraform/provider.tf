terraform {
  required_version = ">= 1.0.0"
  required_providers {
    linode = {
      source  = "linode/linode"
      version = "2.29.1"
    }
    local = {
      source  = "hashicorp/local"
      version = "~> 2.1"
    }
  }
}

provider "linode" {
  token = var.linode_token
}
