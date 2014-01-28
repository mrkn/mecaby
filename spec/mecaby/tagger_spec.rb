require 'spec_helper'

module Mecaby
  describe Tagger do
    subject(:tagger) {
      described_class.new([
        "-d #{dict_dir.join('utf-8')}",
        *additional_args
      ])
    }

    let(:additional_args) { [] }

    describe '.new' do
      context 'When the subject method is called with non-existing dictionary path' do
        subject(:tagger) do
          described_class.new("-d #{dict_dir.join('non-existing-dict')}")
        end

        it 'raises Mecaby::DictionaryNotFound' do
          expect { subject }.to raise_error(Mecaby::DictionaryNotFound)
        end
      end
    end

    describe '#parse' do
      context 'When the tagger is created with "-Oyomi"' do
        let(:additional_args) { [ '-Oyomi' ] }

        context 'the subject method is called with "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.parse(input) }

          it { should eq("タロウトハナコ\n") }
        end
      end

      context 'When the tagger is created with "-Owakati"' do
        let(:additional_args) { [ '-Owakati' ] }

        context 'the subject method is called with "太郎と花子"' do
          let(:input) { "太郎と花子" }
          subject { tagger.parse(input) }

          it { should eq("太郎 と 花子 \n") }
        end
      end
    end
  end
end
