<?php
defined('BASEPATH') OR exit('No direct script access allowed');

class Sms extends CI_Controller {
	public function __construct(){
		parent::__construct();
	}

	///APi codeigniter project in controllers folder
	
	public function api($porta = "",$porta1 = "",$porta2 = ""){
		
		if($porta == "hello" && $porta1 == "1234567" ){
			$row = $this->db->where("status","pending")->get("tbl_sms")->row();
			if($row){
				$message = array();
				$message["id"] = $row->id;
				$message["num"]= $row->phone;
				$message["msg"]= $row->msg;
				echo $message["id"].",".$message["num"].",".$message["msg"];
			}else{
			    echo "invalid";
			}
			return;
		}
		
		if($porta == "success"){
			$input["ddate"] =$porta2;
			$input["status"] = "success";
			$this->db->where("id",$porta1)->update("tbl_sms",$input);
			return;
		}
		
		if($porta == "error"){
			$input["ddate"] = $porta2;
			$input["status"] = "error";
			$this->db->where("id",$porta1)->update("tbl_sms",$input);
			return;
		}
		
	}
	
	public function _404_page(){
		//$data['main_content'] = $this->load->view('front/404','',true);
		$this->session->set_flashdata('msg_error',"Page Not Found!");
		$this->load->view('front/404');
	}
	
///class end
}
