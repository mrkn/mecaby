require 'spec_helper'

module Mecaby
  describe DictionaryInfo do
    context 'with a dictionary encoded in UTF-8' do
      let(:tagger) { Tagger.new("-d #{spec_dir.join('dict/utf-8')}") }
      subject(:dictionary_info) { tagger.dictionary_info }

      describe '#encoding' do
        subject(:encoding) { dictionary_info.encoding }

        it { should eq(Encoding::UTF_8) }
      end
    end

    context 'with a dictionary encoded in Windows-31J' do
      let(:tagger) { Tagger.new("-d #{spec_dir.join('dict/sjis')}") }
      subject(:dictionary_info) { tagger.dictionary_info }

      describe '#encoding' do
        subject(:encoding) { dictionary_info.encoding }

        it { should eq(Encoding::Windows_31J) }
      end
    end

    context 'with a dictionary encoded in EUC-JP' do
      let(:tagger) { Tagger.new("-d #{spec_dir.join('dict/euc-jp')}") }
      subject(:dictionary_info) { tagger.dictionary_info }

      describe '#encoding' do
        subject(:encoding) { dictionary_info.encoding }

        it { should eq(Encoding::EUC_JP) }
      end
    end
  end
end
